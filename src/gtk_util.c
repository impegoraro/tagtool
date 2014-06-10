#include <gtk/gtk.h>

#include "gtk_util.h"


static void cb_enforce_max_chars(GtkEditable *editable, gchar *insert_text, gint insert_text_len, gint *insert_pos, gpointer data)
{
	/* The maximum number of charaters is passed in the signal's user data */
	guint max_len = (guint)data;
	gchar *text = gtk_editable_get_chars(editable, 0, -1);

	if (g_utf8_strlen(text, -1) > max_len)
		gtk_editable_delete_text(editable, max_len, -1);

	g_free(text);
}



/*** public finctions *******************************************************/

gboolean gtk_tree_view_get_first_selected(GtkTreeView *tree, GtkTreeModel **model, GtkTreeIter *iter)
{
	GtkTreeSelection *loc_selection;
	GtkTreeModel *loc_model;

	loc_selection = gtk_tree_view_get_selection(tree);
	loc_model = gtk_tree_view_get_model(tree);

	if (model)
		*model = loc_model;

	if (!iter)
		return gtk_tree_selection_count_selected_rows(loc_selection) > 0;

	if (gtk_tree_selection_get_mode(loc_selection) == GTK_SELECTION_MULTIPLE) {
		GList *selected_rows;
		gboolean result;

		selected_rows = gtk_tree_selection_get_selected_rows(loc_selection, NULL);

		result = gtk_tree_model_get_iter(loc_model, iter, selected_rows->data);

		g_list_foreach(selected_rows, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(selected_rows);

		return result;
	}
	else {
		return gtk_tree_selection_get_selected(loc_selection, NULL, iter);
	}
}


void gtk_editable_set_max_chars(GtkEditable *editable, guint max_chars)
{
	if (max_chars > 0) {
		gtk_signal_connect_after(GTK_OBJECT(editable), "insert_text",
					 GTK_SIGNAL_FUNC(cb_enforce_max_chars),
					 (gpointer)max_chars);
	}
	else {
		gulong id = g_signal_handler_find(GTK_OBJECT(editable), G_SIGNAL_MATCH_FUNC,
						  0, 0, 0, GTK_SIGNAL_FUNC(cb_enforce_max_chars), 0);
		if (id > 0)
			g_signal_handler_disconnect(GTK_OBJECT(editable), id);
	}
}


