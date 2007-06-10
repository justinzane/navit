struct map_methods;

struct maptype {
	char *name;
	struct map_priv *(*map_new)(struct map_methods *meth, char *data);
	struct maptype *next;	
};

/* prototypes */
struct map_methods;
struct map_priv;
struct maptype;
void maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data));
struct maptype *maptype_get(const char *name);
