PLUGIN_FUNC1(draw, struct container *, co)
PLUGIN_FUNC3(popup, struct container *, map, struct popup *, p, struct popup_item **, list)
PLUGIN_TYPE(map, (struct map_methods *meth, char *data)) 
enum window_flags;
PLUGIN_TYPE(gui, (struct gui_methods *meth, int w, int h)) 
PLUGIN_TYPE(graphics, (struct graphics_methods *meth)) 
