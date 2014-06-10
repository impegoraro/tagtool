#ifndef GTK_UTIL_H
#define GTK_UTIL_H


/*
 * Sets <iter> to the currently selected row. If there is more than one 
 * row selected returns the first one.
 * 
 * <tree>	The tree view
 * <model>	(output) If not NULL, is set to the tree view's model.
 * <iter>	(output) If not NULL, is set to the first selected row.
 *
 * return	TRUE if there is a selected row, FALSE otherwise.
 */
gboolean gtk_tree_view_get_first_selected( GtkTreeView *tree, 
					   GtkTreeModel **model, 
					   GtkTreeIter *iter );


/*
 * Sets the maximum number of characters the GtkEditable will accept.
 * 
 * <editable>	A GtkEditable widget.
 * <max_chars>	Maximum number of characters. 0 means no limit.
 */
void gtk_editable_set_max_chars( GtkEditable *editable, 
				 guint max_chars );


#endif
