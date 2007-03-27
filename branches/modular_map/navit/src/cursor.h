/* prototypes */
struct color;
struct cursor;
struct graphics;
struct transformation;
struct vehicle;
struct cursor *cursor_new(struct graphics *gra, struct vehicle *v, struct color *c, struct transformation *t);
