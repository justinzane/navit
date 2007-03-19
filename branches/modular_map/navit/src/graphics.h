
struct point;
struct container;
struct color;
struct graphics;
struct graphics_gc;
struct graphics_font;
struct graphics_image;

void graphics_get_view(struct container *co, long *x, long *y, unsigned long *scale);
void graphics_set_view(struct container *co, long *x, long *y, unsigned long *scale);
void graphics_resize(struct container *co, int w, int h);
void graphics_redraw(struct container *co);
struct graphics_font *graphics_font_new(struct graphics *gra, int size);
struct graphics_gc *graphics_gc_new(struct graphics *gra);

enum draw_mode_num {
	draw_mode_begin, draw_mode_end, draw_mode_cursor
};

struct graphics_priv;
struct graphics_font_priv;
struct graphics_image_priv;
struct graphics_gc_priv;
struct graphics_font_methods;
struct graphics_gc_methods;
struct graphics_image_methods;

struct graphics_methods {
	void (*graphics_destroy)(struct graphics_priv *gr);
	void (*draw_mode)(struct graphics_priv *gr, enum draw_mode_num mode);
	void (*draw_lines)(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count);
	void (*draw_polygon)(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count);
	void (*draw_rectangle)(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h);
	void (*draw_circle)(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r);
	void (*draw_text)(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy);
	void (*draw_image)(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img);
	void (*draw_restore)(struct graphics_priv *gr, struct point *p, int w, int h);
	struct graphics_font_priv *(*font_new)(struct graphics_priv *gr, struct graphics_font_methods *meth, int size);
	struct graphics_gc_priv *(*gc_new)(struct graphics_priv *gr, struct graphics_gc_methods *meth);
	struct graphics_priv *(*overlay_new)(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h);
	struct graphics_image_priv *(*image_new)(struct graphics_priv *gr, struct graphics_image_methods *meth, char *path, int *w, int *h);
	void *(*get_data)(struct graphics_priv *gr, char *type);
	void (*register_resize_callback)(struct graphics_priv *gr, void (*callback)(void *data, int w, int h), void *data);
	void (*background_gc)(struct graphics_priv *gr, struct graphics_gc_priv *gc);
};

struct graphics
{
	struct graphics_priv *priv;
	struct graphics_methods meth;
	struct graphics_font *font[3];
	struct graphics_gc *gc[3];

};

struct graphics_font_methods {
	void (*font_destroy)(struct graphics_font_priv *font);
};

struct graphics_font {
	struct graphics_font_priv *priv;
	struct graphics_font_methods meth;
};

struct graphics_gc_methods {
	void (*gc_destroy)(struct graphics_gc_priv *gc);
	void (*gc_set_linewidth)(struct graphics_gc_priv *gc, int width);
	void (*gc_set_dashes)(struct graphics_gc_priv *gc, unsigned char dash_list[], int n);
	void (*gc_set_foreground)(struct graphics_gc_priv *gc, struct color *c);
	void (*gc_set_background)(struct graphics_gc_priv *gc, struct color *c);
};

struct graphics_gc {
	struct graphics_gc_priv *priv;
	struct graphics_gc_methods meth;
};

struct graphics_image_methods {
	void (*image_destroy)(struct graphics_image_priv *img);
};

struct graphics_image {
	struct graphics_image_priv *priv;
	struct graphics_image_methods meth;
	int width;
	int height;
};


