#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "param.h"
#include "data_window.h"
#include "data_window_int.h"

struct data_window *
data_window(char *name, struct window *parent, void(*callback)(struct data_window *, char **cols))
{
	struct data_window *win;

	win=malloc(sizeof(*win));
	win->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win->window), 320, 200);
	gtk_window_set_title(GTK_WINDOW(win->window), name);

	win->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win->scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(win->window), win->scrolled_window);

	win->treeview=NULL;
	win->callback=callback;
	if (parent) 
		gtk_window_set_transient_for(GTK_WINDOW((GtkWidget *)(win->window)), GTK_WINDOW(parent));
	gtk_widget_show_all(win->window);
	return win;
}

void
data_window_begin(struct data_window *win)
{
	if (win && win->treeview) {
		gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), NULL);
	}
}

static void 
click_column(GtkCList *clist, int column)
{
	if (column != clist->sort_column) {
		gtk_clist_set_sort_type(clist, GTK_SORT_ASCENDING);
		gtk_clist_set_sort_column(clist, column);
	} else {
		if (clist->sort_type == GTK_SORT_ASCENDING)
			gtk_clist_set_sort_type(clist, GTK_SORT_DESCENDING);
		else
			gtk_clist_set_sort_type(clist, GTK_SORT_ASCENDING);
	}
	gtk_clist_sort(clist);
}

		GValue value;
static void 
select_row(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, struct data_window *win)
{
	int i;
	if (win->callback) {
		printf("callback\n");
		
		char *cols[20];
		GtkTreeIter iter;
		GtkTreeModel *model;

		model=gtk_tree_view_get_model(tree);
		gtk_tree_model_get_iter(model, &iter, path);

		for (i=0;i<gtk_tree_model_get_n_columns(model);i++) {
			gtk_tree_model_get_value(model, &iter, i, &value);
			cols[i]=g_strdup_value_contents(&value)+1;
			cols[i][strlen(cols[i])-1]='\0';
			g_value_unset(&value);
		}	
		win->callback(win, cols);
	}
}

void
data_window_add(struct data_window *win, struct param_list *param, int count)
{
	int i;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GtkListStore *liststore;
	GType types[count];
	gchar *utf8;

	if (! win->treeview) {
		win->treeview=gtk_tree_view_new();
		gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), NULL);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(win->scrolled_window),win->treeview);
		gtk_widget_show_all(GTK_WIDGET(win->window));
		/* add column names to treeview */
		for(i=0;i<count;i++) {
			cell=gtk_cell_renderer_text_new();
			gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (win->treeview),-1,param[i].name,
					cell,"text",i, NULL);
		}
#if 0
		g_signal_connect(G_OBJECT(win->treeview), "click-column", G_CALLBACK(click_column), NULL);
#endif
		g_signal_connect(G_OBJECT(win->treeview), "row-activated", G_CALLBACK(select_row), win);
	}

	/* find data storage and create a new one if none is there */
	if (gtk_tree_view_get_model(GTK_TREE_VIEW (win->treeview)) == NULL) {
		for(i=0;i<count;i++) types[i]=G_TYPE_STRING;
	    	liststore=gtk_list_store_newv(count,types);
		gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), GTK_TREE_MODEL(liststore));
	}
	else
		liststore=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (win->treeview)));

	gtk_list_store_append(liststore,&iter);

	/* add data to data storage */
	for(i=0;i<count;i++) {
		utf8=g_locale_to_utf8(param[i].value,-1,NULL,NULL,NULL);
		gtk_list_store_set(liststore,&iter,i,utf8,-1);
	}

}

void
data_window_end(struct data_window *win)
{
}
