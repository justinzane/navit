struct maptype {
	char *name;
	struct map_priv *(*map_new)(struct map_methods *meth, char *data);
	struct maptype *next;	
};

void maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data));
struct maptype * maptype_get(char *name);
