#include <glib.h>
#include <glib/gprintf.h>
#include "container.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "layout.h"


struct elem_data {
	struct container *co;
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

	g_printf("start_element: %s AN: %s AV: %s\n",element_name,*attribute_names,*attribute_values);


	if(!g_ascii_strcasecmp("navit", element_name)) {
		if (parent(parent_token, NULL, error))
			elem = data->co;
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
			elem = map_new(find_attribute("type", attribute_names, attribute_values),
					find_attribute("data", attribute_names, attribute_values));
			mapset_add(ms, elem);
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
		struct layout *layer=parent_object;
		if(parent(parent_token, "layer", error)) {
			elem =item_new(find_attribute("type", attribute_names, attribute_values),
					convert_number(find_attribute("zoom",attribute_names, attribute_values)));
			layer_add_item(layer, elem);
		}
	}
	else  {
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

	g_printf("end_element: %s\n",element_name);

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


gboolean config_load(char *filename, struct container *co, GError **error)
{
	GMarkupParseContext *context;
	char *contents;
	gsize len;
	gboolean result;

	struct elem_data data;
	
	data.co = co;
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

