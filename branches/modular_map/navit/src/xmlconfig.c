#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include "container.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "layout.h"


struct elem_data {
	GList *elem_stack;
	GList *token_stack;
};


static char * find_attribute(const char *attribute, const char **attribute_name, const char **attribute_value)
{
	while(*attribute_name) {
		if(! g_ascii_strcasecmp(attribute,*attribute_name))
			return g_strdup(*attribute_value);
		attribute_name++;
		attribute_value++;
	}
	return NULL;
}

static int
find_color(const char **attribute_name, const char **attribute_value, int *color)
{
	char *value;

	value=find_attribute("color", attribute_name, attribute_value);
	if (! value)
		return 0;

	sscanf(value,"#%02x%02x%02x", color, color+1, color+2);
	color[0] = (color[0] << 8) | color[0];
	color[1] = (color[1] << 8) | color[1];
	color[2] = (color[2] << 8) | color[2];
	return 1;
}

static int
find_zoom(const char **attribute_name, const char **attribute_value, int *min, int *max)
{
	char *value, *pos;
	int ret;

	*min=0;
	*max=15;
	value=find_attribute("zoom", attribute_name, attribute_value);
	if (! value)
		return 0;
	pos=index(value, '-');
	if (! pos) {
		ret=sscanf(value,"%d",min);
		*max=*min;
	} else if (pos == value) 
		ret=sscanf(value,"-%d",max);
	else
		ret=sscanf(value,"%d-%d", min, max);
	return ret;
}

static int
convert_number(const char *val)
{
	return g_ascii_strtoull(val,NULL,0);
}

static int
parent(char *actual, char *required, GError **error)
{
	if (! actual && ! required)
		return 1;
	if (! actual || ! required || g_ascii_strcasecmp(actual, required) ) {
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
				"Element '%s' within unexpected context. Expected '%s'",
				actual, required);
		return 0;
	}
	return 1;
}

#if 0
static struct vehile_data * config_add_vehicle(struct config_data *config, char *name, char *source, char *color)
{
	struct vehicle_data *v;

	v = g_new0(struct vehicle_data,1);
	v->name = name;
	v->source = source;
	v->color = color;
	config->vehicles =
		g_list_append(config->vehicles, v);
}
#endif

static void
start_element (GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		gpointer             user_data,
		GError             **error)
{
	struct elem_data *data = user_data;
	void *elem=NULL;
	void *parent_object;
	char *parent_token;
	parent_object=data->elem_stack ? data->elem_stack->data : NULL;
	parent_token=data->token_stack ? data->token_stack->data : NULL;

	/* g_printf("start_element: %s AN: %s AV: %s\n",element_name,*attribute_names,*attribute_values); */


	if(!g_ascii_strcasecmp("navit", element_name)) {
		if (parent(parent_token, NULL, error)) {
			elem = navit_new();
		}
	}
	else if(!g_ascii_strcasecmp("vehicle", element_name)) {
#if 0
		elem = config_add_vehicle(data->config,
				find_attribute("name", attribute_names, attribute_values),
				find_attribute("source", attribute_names, attribute_values),
				find_attribute("color", attribute_names, attribute_values));
#endif
	}
	else if(!g_ascii_strcasecmp("mapset", element_name)) {
		struct container *co=parent_object;
		if (parent(parent_token, "navit", error)) {
			elem = mapset_new();
			co->mapsets = g_list_append(co->mapsets, elem);
		}
	}
	else if(!g_ascii_strcasecmp("map", element_name)) {
		struct mapset *ms=parent_object;
		if (parent(parent_token, "mapset", error)) {
			char *enabled;
			enabled=find_attribute("enabled", attribute_names, attribute_values);
			if (! enabled || (g_ascii_strcasecmp(enabled,"no") && g_ascii_strcasecmp(enabled,"0"))) {
				elem = map_new(find_attribute("type", attribute_names, attribute_values),
						find_attribute("data", attribute_names, attribute_values));
				mapset_add(ms, elem);
			}
		}
	}
	else if(!g_ascii_strcasecmp("layout", element_name)) {
		struct container *co=parent_object;
		if(parent(parent_token, "navit", error)) {
			elem =layout_new(find_attribute("name", attribute_names, attribute_values));
			co->layouts = g_list_append(co->layouts, elem);
		}
	}
	else if(!g_ascii_strcasecmp("layer", element_name)) {
		struct layout *layout=parent_object;
		if(parent(parent_token, "layout", error)) {
			elem =layer_new(find_attribute("name", attribute_names, attribute_values),
					convert_number(find_attribute("details",attribute_names, attribute_values)));
			layout_add_layer(layout, elem);
		}
	}
	else if(!g_ascii_strcasecmp("item", element_name)) {
		struct layer *layer=parent_object;
		if(parent(parent_token, "layer", error)) {
			char *s=g_strdup(find_attribute("type", attribute_names, attribute_values));
			int min, max;
			enum item_type type;
			if (find_zoom(attribute_names, attribute_values, &min, &max)) {
				char *saveptr, *tok, *str=s;
				elem=itemtype_new(min, max);
				layer_add_itemtype(layer, elem);
				while ((tok=strtok_r(str, ",", &saveptr))) {
					type=item_from_name(tok);
					itemtype_add_type(elem, type);
					str=NULL;
				}
				g_free(s);
			}
		}
	}
	else if(!g_ascii_strcasecmp("polygon", element_name) || !g_ascii_strcasecmp("polyline", element_name) || !g_ascii_strcasecmp("circle", element_name)) {
		struct itemtype *itm=parent_object;
		if(parent(parent_token, "item", error)) {
			int color[3];
			struct element *e=NULL;
			if (find_color(attribute_names, attribute_values, color)) {
				int w=0;
				if (!g_ascii_strcasecmp("polyline", element_name) || !g_ascii_strcasecmp("circle", element_name)) {
					char *s=find_attribute("width",attribute_names, attribute_values);
					if (s) 
						w=convert_number(s);
				}
				if (!g_ascii_strcasecmp("polygon", element_name)) {
					e=polygon_new(color);
				}
				if (!g_ascii_strcasecmp("polyline", element_name)) {
					e=polyline_new(color, w);
				}
				if (!g_ascii_strcasecmp("circle", element_name)) {
					int r=0,ls=0;
					char *s;
					s=find_attribute("radius",attribute_names, attribute_values);
					if (s) 
						r=convert_number(s);
					s=find_attribute("label_size",attribute_names, attribute_values);
					if (s) 
						ls=convert_number(s);
					e=circle_new(color, r, w, ls);
				}
				itemtype_add_element(itm, e);
			}
		}
	}
	else if(!g_ascii_strcasecmp("icon", element_name)) {
		struct itemtype *itm=parent_object;
		if(parent(parent_token, "item", error)) {
			struct element *e;
			char *src;
			src=find_attribute("src",attribute_names, attribute_values);
			if (src) {
				e=icon_new(src);
				itemtype_add_element(itm, e);
			}
		}	
	}
	else  {
		printf("Unknown element '%s'\n", element_name);
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
				"Unknown element '%s'", element_name);
	}
	data->elem_stack = g_list_prepend(data->elem_stack, elem);
	data->token_stack = g_list_prepend(data->token_stack, (gpointer)element_name);
}


/* Called for close tags </foo> */
static void
end_element (GMarkupParseContext *context,
		const gchar         *element_name,
		gpointer             user_data,
		GError             **error)
{
	struct elem_data *data = user_data;

	/* g_printf("end_element: %s\n",element_name); */

	data->token_stack = g_list_delete_link (data->token_stack, data->token_stack);
	data->elem_stack = g_list_delete_link (data->elem_stack, data->elem_stack);

}


/* Called for character data */
/* text is not nul-terminated */
static void
text (GMarkupParseContext *context,
		const gchar            *text,
		gsize                   text_len,
		gpointer                user_data,
		GError               **error)
{
	struct elem_data *data = user_data;

	(void) data;
}



static const GMarkupParser parser = {
	start_element,
	end_element,
	text,
	NULL,
	NULL
};


gboolean config_load(char *filename, GError **error)
{
	GMarkupParseContext *context;
	char *contents;
	gsize len;
	gboolean result;

	struct elem_data data;
	
	data.elem_stack = NULL;
	data.token_stack = NULL;
	

	context = g_markup_parse_context_new (&parser, 0, &data, NULL);

	if (!g_file_get_contents (filename, &contents, &len, error))
		return FALSE;

	result = g_markup_parse_context_parse (context, contents, len, error);

	g_markup_parse_context_free (context);
	g_free (contents);

	return result;
}

